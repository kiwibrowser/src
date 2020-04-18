// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/test/fake_central.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/location.h"
#include "base/task_scheduler/post_task.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_discovery_filter.h"
#include "device/bluetooth/bluetooth_discovery_session_outcome.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "device/bluetooth/public/mojom/test/fake_bluetooth.mojom.h"
#include "device/bluetooth/test/fake_peripheral.h"
#include "device/bluetooth/test/fake_remote_gatt_characteristic.h"
#include "device/bluetooth/test/fake_remote_gatt_service.h"

namespace bluetooth {

FakeCentral::FakeCentral(mojom::CentralState state,
                         mojom::FakeCentralRequest request)
    : has_pending_or_active_discovery_session_(false),
      state_(state),
      binding_(this, std::move(request)) {}

void FakeCentral::SimulatePreconnectedPeripheral(
    const std::string& address,
    const std::string& name,
    const std::vector<device::BluetoothUUID>& known_service_uuids,
    SimulatePreconnectedPeripheralCallback callback) {
  FakePeripheral* fake_peripheral = GetFakePeripheral(address);
  if (fake_peripheral == nullptr) {
    auto fake_peripheral_ptr = std::make_unique<FakePeripheral>(this, address);
    fake_peripheral = fake_peripheral_ptr.get();
    auto pair = devices_.emplace(address, std::move(fake_peripheral_ptr));
    DCHECK(pair.second);
  }

  fake_peripheral->SetName(name);
  fake_peripheral->SetSystemConnected(true);
  fake_peripheral->SetServiceUUIDs(device::BluetoothDevice::UUIDSet(
      known_service_uuids.begin(), known_service_uuids.end()));

  std::move(callback).Run();
}

void FakeCentral::SimulateAdvertisementReceived(
    mojom::ScanResultPtr scan_result_ptr,
    SimulateAdvertisementReceivedCallback callback) {
  // TODO(https://crbug.com/719826): Add a DCHECK to proceed only if a scan is
  // currently in progress.
  auto* fake_peripheral = GetFakePeripheral(scan_result_ptr->device_address);
  const bool is_new_device = fake_peripheral == nullptr;
  if (is_new_device) {
    auto fake_peripheral_ptr =
        std::make_unique<FakePeripheral>(this, scan_result_ptr->device_address);
    fake_peripheral = fake_peripheral_ptr.get();
    auto pair = devices_.emplace(scan_result_ptr->device_address,
                                 std::move(fake_peripheral_ptr));
    DCHECK(pair.second);
  }

  if (scan_result_ptr->scan_record->name) {
    fake_peripheral->SetName(scan_result_ptr->scan_record->name.value());
  }

  device::BluetoothDevice::UUIDList uuids =
      (scan_result_ptr->scan_record->uuids)
          ? device::BluetoothDevice::UUIDList(
                scan_result_ptr->scan_record->uuids.value().begin(),
                scan_result_ptr->scan_record->uuids.value().end())
          : device::BluetoothDevice::UUIDList();
  device::BluetoothDevice::ServiceDataMap service_data =
      (scan_result_ptr->scan_record->service_data)
          ? device::BluetoothDevice::ServiceDataMap(
                scan_result_ptr->scan_record->service_data.value().begin(),
                scan_result_ptr->scan_record->service_data.value().end())
          : device::BluetoothDevice::ServiceDataMap();
  device::BluetoothDevice::ManufacturerDataMap manufacturer_data =
      (scan_result_ptr->scan_record->manufacturer_data)
          ? device::BluetoothDevice::ManufacturerDataMap(
                scan_result_ptr->scan_record->manufacturer_data.value().begin(),
                scan_result_ptr->scan_record->manufacturer_data.value().end())
          : device::BluetoothDevice::ManufacturerDataMap();

  fake_peripheral->UpdateAdvertisementData(
      scan_result_ptr->rssi, std::move(uuids), std::move(service_data),
      std::move(manufacturer_data),
      (scan_result_ptr->scan_record->tx_power->has_value)
          ? &scan_result_ptr->scan_record->tx_power->value
          : nullptr);

  if (is_new_device) {
    // Call DeviceAdded on observers because it is a newly detected peripheral.
    for (auto& observer : observers_) {
      observer.DeviceAdded(this, fake_peripheral);
    }
  } else {
    // Call DeviceChanged on observers because it is a device that was detected
    // before.
    for (auto& observer : observers_) {
      observer.DeviceChanged(this, fake_peripheral);
    }
  }

  std::move(callback).Run();
}

void FakeCentral::SetNextGATTConnectionResponse(
    const std::string& address,
    uint16_t code,
    SetNextGATTConnectionResponseCallback callback) {
  FakePeripheral* fake_peripheral = GetFakePeripheral(address);
  if (fake_peripheral == nullptr) {
    std::move(callback).Run(false);
    return;
  }

  fake_peripheral->SetNextGATTConnectionResponse(code);
  std::move(callback).Run(true);
}

void FakeCentral::SetNextGATTDiscoveryResponse(
    const std::string& address,
    uint16_t code,
    SetNextGATTDiscoveryResponseCallback callback) {
  FakePeripheral* fake_peripheral = GetFakePeripheral(address);
  if (fake_peripheral == nullptr) {
    std::move(callback).Run(false);
    return;
  }

  fake_peripheral->SetNextGATTDiscoveryResponse(code);
  std::move(callback).Run(true);
}

bool FakeCentral::AllResponsesConsumed() {
  return std::all_of(devices_.begin(), devices_.end(), [](const auto& e) {
    // static_cast is safe because the parent class's devices_ is only
    // populated via this FakeCentral, and only with FakePeripherals.
    FakePeripheral* fake_peripheral =
        static_cast<FakePeripheral*>(e.second.get());
    return fake_peripheral->AllResponsesConsumed();
  });
}

void FakeCentral::SimulateGATTDisconnection(
    const std::string& address,
    SimulateGATTDisconnectionCallback callback) {
  FakePeripheral* fake_peripheral = GetFakePeripheral(address);
  if (fake_peripheral == nullptr) {
    std::move(callback).Run(false);
  }

  fake_peripheral->SimulateGATTDisconnection();
  std::move(callback).Run(true);
}

void FakeCentral::SimulateGATTServicesChanged(
    const std::string& address,
    SimulateGATTServicesChangedCallback callback) {
  FakePeripheral* fake_peripheral = GetFakePeripheral(address);
  if (fake_peripheral == nullptr) {
    std::move(callback).Run(false);
    return;
  }

  std::move(callback).Run(true);
}

void FakeCentral::AddFakeService(const std::string& peripheral_address,
                                 const device::BluetoothUUID& service_uuid,
                                 AddFakeServiceCallback callback) {
  FakePeripheral* fake_peripheral = GetFakePeripheral(peripheral_address);
  if (fake_peripheral == nullptr) {
    std::move(callback).Run(base::nullopt);
    return;
  }

  std::move(callback).Run(fake_peripheral->AddFakeService(service_uuid));
}

void FakeCentral::RemoveFakeService(const std::string& identifier,
                                    const std::string& peripheral_address,
                                    RemoveFakeServiceCallback callback) {
  FakePeripheral* fake_peripheral = GetFakePeripheral(peripheral_address);
  if (!fake_peripheral) {
    std::move(callback).Run(false);
    return;
  }
  std::move(callback).Run(fake_peripheral->RemoveFakeService(identifier));
}

void FakeCentral::AddFakeCharacteristic(
    const device::BluetoothUUID& characteristic_uuid,
    mojom::CharacteristicPropertiesPtr properties,
    const std::string& service_id,
    const std::string& peripheral_address,
    AddFakeCharacteristicCallback callback) {
  FakeRemoteGattService* fake_remote_gatt_service =
      GetFakeRemoteGattService(peripheral_address, service_id);
  if (fake_remote_gatt_service == nullptr) {
    std::move(callback).Run(base::nullopt);
    return;
  }

  std::move(callback).Run(fake_remote_gatt_service->AddFakeCharacteristic(
      characteristic_uuid, std::move(properties)));
}

void FakeCentral::RemoveFakeCharacteristic(
    const std::string& identifier,
    const std::string& service_id,
    const std::string& peripheral_address,
    RemoveFakeCharacteristicCallback callback) {
  FakeRemoteGattService* fake_remote_gatt_service =
      GetFakeRemoteGattService(peripheral_address, service_id);
  if (fake_remote_gatt_service == nullptr) {
    std::move(callback).Run(false);
    return;
  }

  std::move(callback).Run(
      fake_remote_gatt_service->RemoveFakeCharacteristic(identifier));
}

void FakeCentral::AddFakeDescriptor(
    const device::BluetoothUUID& descriptor_uuid,
    const std::string& characteristic_id,
    const std::string& service_id,
    const std::string& peripheral_address,
    AddFakeDescriptorCallback callback) {
  FakeRemoteGattCharacteristic* fake_remote_gatt_characteristic =
      GetFakeRemoteGattCharacteristic(peripheral_address, service_id,
                                      characteristic_id);
  if (fake_remote_gatt_characteristic == nullptr) {
    std::move(callback).Run(base::nullopt);
  }

  std::move(callback).Run(
      fake_remote_gatt_characteristic->AddFakeDescriptor(descriptor_uuid));
}

void FakeCentral::RemoveFakeDescriptor(const std::string& descriptor_id,
                                       const std::string& characteristic_id,
                                       const std::string& service_id,
                                       const std::string& peripheral_address,
                                       RemoveFakeDescriptorCallback callback) {
  FakeRemoteGattCharacteristic* fake_remote_gatt_characteristic =
      GetFakeRemoteGattCharacteristic(peripheral_address, service_id,
                                      characteristic_id);
  if (!fake_remote_gatt_characteristic) {
    std::move(callback).Run(false);
    return;
  }

  std::move(callback).Run(
      fake_remote_gatt_characteristic->RemoveFakeDescriptor(descriptor_id));
}

void FakeCentral::SetNextReadCharacteristicResponse(
    uint16_t gatt_code,
    const base::Optional<std::vector<uint8_t>>& value,
    const std::string& characteristic_id,
    const std::string& service_id,
    const std::string& peripheral_address,
    SetNextReadCharacteristicResponseCallback callback) {
  FakeRemoteGattCharacteristic* fake_remote_gatt_characteristic =
      GetFakeRemoteGattCharacteristic(peripheral_address, service_id,
                                      characteristic_id);
  if (fake_remote_gatt_characteristic == nullptr) {
    std::move(callback).Run(false);
  }

  fake_remote_gatt_characteristic->SetNextReadResponse(gatt_code, value);
  std::move(callback).Run(true);
}

void FakeCentral::SetNextWriteCharacteristicResponse(
    uint16_t gatt_code,
    const std::string& characteristic_id,
    const std::string& service_id,
    const std::string& peripheral_address,
    SetNextWriteCharacteristicResponseCallback callback) {
  FakeRemoteGattCharacteristic* fake_remote_gatt_characteristic =
      GetFakeRemoteGattCharacteristic(peripheral_address, service_id,
                                      characteristic_id);
  if (fake_remote_gatt_characteristic == nullptr) {
    std::move(callback).Run(false);
  }

  fake_remote_gatt_characteristic->SetNextWriteResponse(gatt_code);
  std::move(callback).Run(true);
}

void FakeCentral::SetNextSubscribeToNotificationsResponse(
    uint16_t gatt_code,
    const std::string& characteristic_id,
    const std::string& service_id,
    const std::string& peripheral_address,
    SetNextSubscribeToNotificationsResponseCallback callback) {
  FakeRemoteGattCharacteristic* fake_remote_gatt_characteristic =
      GetFakeRemoteGattCharacteristic(peripheral_address, service_id,
                                      characteristic_id);
  if (fake_remote_gatt_characteristic == nullptr) {
    std::move(callback).Run(false);
  }

  fake_remote_gatt_characteristic->SetNextSubscribeToNotificationsResponse(
      gatt_code);
  std::move(callback).Run(true);
}

void FakeCentral::SetNextUnsubscribeFromNotificationsResponse(
    uint16_t gatt_code,
    const std::string& characteristic_id,
    const std::string& service_id,
    const std::string& peripheral_address,
    SetNextUnsubscribeFromNotificationsResponseCallback callback) {
  FakeRemoteGattCharacteristic* fake_remote_gatt_characteristic =
      GetFakeRemoteGattCharacteristic(peripheral_address, service_id,
                                      characteristic_id);
  if (fake_remote_gatt_characteristic == nullptr) {
    std::move(callback).Run(false);
  }

  fake_remote_gatt_characteristic->SetNextUnsubscribeFromNotificationsResponse(
      gatt_code);
  std::move(callback).Run(true);
}

void FakeCentral::IsNotifying(const std::string& characteristic_id,
                              const std::string& service_id,
                              const std::string& peripheral_address,
                              IsNotifyingCallback callback) {
  FakeRemoteGattCharacteristic* fake_remote_gatt_characteristic =
      GetFakeRemoteGattCharacteristic(peripheral_address, service_id,
                                      characteristic_id);
  if (!fake_remote_gatt_characteristic) {
    std::move(callback).Run(false, false);
  }

  std::move(callback).Run(true, fake_remote_gatt_characteristic->IsNotifying());
}

void FakeCentral::GetLastWrittenCharacteristicValue(
    const std::string& characteristic_id,
    const std::string& service_id,
    const std::string& peripheral_address,
    GetLastWrittenCharacteristicValueCallback callback) {
  FakeRemoteGattCharacteristic* fake_remote_gatt_characteristic =
      GetFakeRemoteGattCharacteristic(peripheral_address, service_id,
                                      characteristic_id);
  if (fake_remote_gatt_characteristic == nullptr) {
    std::move(callback).Run(false, base::nullopt);
  }

  std::move(callback).Run(
      true, fake_remote_gatt_characteristic->last_written_value());
}

void FakeCentral::SetNextReadDescriptorResponse(
    uint16_t gatt_code,
    const base::Optional<std::vector<uint8_t>>& value,
    const std::string& descriptor_id,
    const std::string& characteristic_id,
    const std::string& service_id,
    const std::string& peripheral_address,
    SetNextReadDescriptorResponseCallback callback) {
  FakeRemoteGattDescriptor* fake_remote_gatt_descriptor =
      GetFakeRemoteGattDescriptor(peripheral_address, service_id,
                                  characteristic_id, descriptor_id);
  if (fake_remote_gatt_descriptor == nullptr) {
    std::move(callback).Run(false);
  }

  fake_remote_gatt_descriptor->SetNextReadResponse(gatt_code, value);
  std::move(callback).Run(true);
}

void FakeCentral::SetNextWriteDescriptorResponse(
    uint16_t gatt_code,
    const std::string& descriptor_id,
    const std::string& characteristic_id,
    const std::string& service_id,
    const std::string& peripheral_address,
    SetNextWriteDescriptorResponseCallback callback) {
  FakeRemoteGattDescriptor* fake_remote_gatt_descriptor =
      GetFakeRemoteGattDescriptor(peripheral_address, service_id,
                                  characteristic_id, descriptor_id);
  if (!fake_remote_gatt_descriptor) {
    std::move(callback).Run(false);
  }

  fake_remote_gatt_descriptor->SetNextWriteResponse(gatt_code);
  std::move(callback).Run(true);
}

void FakeCentral::GetLastWrittenDescriptorValue(
    const std::string& descriptor_id,
    const std::string& characteristic_id,
    const std::string& service_id,
    const std::string& peripheral_address,
    GetLastWrittenDescriptorValueCallback callback) {
  FakeRemoteGattDescriptor* fake_remote_gatt_descriptor =
      GetFakeRemoteGattDescriptor(peripheral_address, service_id,
                                  characteristic_id, descriptor_id);
  if (!fake_remote_gatt_descriptor) {
    std::move(callback).Run(false, base::nullopt);
  }

  std::move(callback).Run(true,
                          fake_remote_gatt_descriptor->last_written_value());
}

std::string FakeCentral::GetAddress() const {
  NOTREACHED();
  return std::string();
}

std::string FakeCentral::GetName() const {
  NOTREACHED();
  return std::string();
}

void FakeCentral::SetName(const std::string& name,
                          const base::Closure& callback,
                          const ErrorCallback& error_callback) {
  NOTREACHED();
}

bool FakeCentral::IsInitialized() const {
  return true;
}

bool FakeCentral::IsPresent() const {
  switch (state_) {
    case mojom::CentralState::ABSENT:
      return false;
    case mojom::CentralState::POWERED_OFF:
    case mojom::CentralState::POWERED_ON:
      return true;
  }
  NOTREACHED();
  return false;
}

bool FakeCentral::IsPowered() const {
  switch (state_) {
    case mojom::CentralState::POWERED_OFF:
      return false;
    case mojom::CentralState::POWERED_ON:
      return true;
    case mojom::CentralState::ABSENT:
      // Clients shouldn't call IsPowered() when the adapter is not present.
      NOTREACHED();
      return false;
  }
  NOTREACHED();
  return false;
}

void FakeCentral::SetPowered(bool powered,
                             const base::Closure& callback,
                             const ErrorCallback& error_callback) {
  NOTREACHED();
}

bool FakeCentral::IsDiscoverable() const {
  NOTREACHED();
  return false;
}

void FakeCentral::SetDiscoverable(bool discoverable,
                                  const base::Closure& callback,
                                  const ErrorCallback& error_callback) {
  NOTREACHED();
}

bool FakeCentral::IsDiscovering() const {
  NOTREACHED();
  return false;
}

FakeCentral::UUIDList FakeCentral::GetUUIDs() const {
  NOTREACHED();
  return UUIDList();
}

void FakeCentral::CreateRfcommService(
    const device::BluetoothUUID& uuid,
    const ServiceOptions& options,
    const CreateServiceCallback& callback,
    const CreateServiceErrorCallback& error_callback) {
  NOTREACHED();
}

void FakeCentral::CreateL2capService(
    const device::BluetoothUUID& uuid,
    const ServiceOptions& options,
    const CreateServiceCallback& callback,
    const CreateServiceErrorCallback& error_callback) {
  NOTREACHED();
}

void FakeCentral::RegisterAdvertisement(
    std::unique_ptr<device::BluetoothAdvertisement::Data> advertisement_data,
    const CreateAdvertisementCallback& callback,
    const AdvertisementErrorCallback& error_callback) {
  NOTREACHED();
}

#if defined(OS_CHROMEOS) || defined(OS_LINUX)
void FakeCentral::SetAdvertisingInterval(
    const base::TimeDelta& min,
    const base::TimeDelta& max,
    const base::Closure& callback,
    const AdvertisementErrorCallback& error_callback) {
  NOTREACHED();
}
void FakeCentral::ResetAdvertising(
    const base::Closure& callback,
    const AdvertisementErrorCallback& error_callback) {
  NOTREACHED();
}
#endif

device::BluetoothLocalGattService* FakeCentral::GetGattService(
    const std::string& identifier) const {
  NOTREACHED();
  return nullptr;
}

bool FakeCentral::SetPoweredImpl(bool powered) {
  NOTREACHED();
  return false;
}

void FakeCentral::AddDiscoverySession(
    device::BluetoothDiscoveryFilter* discovery_filter,
    const base::Closure& callback,
    DiscoverySessionErrorCallback error_callback) {
  if (!IsPresent()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(
            std::move(error_callback),
            device::UMABluetoothDiscoverySessionOutcome::ADAPTER_NOT_PRESENT));
    return;
  }

  // TODO(https://crbug.com/820113): Currently, multiple discovery sessions are
  // not supported, so we DCHECK to ensure that there is not an active session
  // already.
  DCHECK(!has_pending_or_active_discovery_session_);
  has_pending_or_active_discovery_session_ = true;
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                base::BindOnce(callback));
}

void FakeCentral::RemoveDiscoverySession(
    device::BluetoothDiscoveryFilter* discovery_filter,
    const base::Closure& callback,
    DiscoverySessionErrorCallback error_callback) {
  if (!IsPresent()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(
            std::move(error_callback),
            device::UMABluetoothDiscoverySessionOutcome::ADAPTER_NOT_PRESENT));
    return;
  }

  // TODO(https://crbug.com/820113): Currently, multiple discovery sessions are
  // not supported, so we DCHECK to ensure that there is currently an active
  // session already.
  DCHECK(has_pending_or_active_discovery_session_);
  has_pending_or_active_discovery_session_ = false;
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                base::BindOnce(callback));
}

void FakeCentral::SetDiscoveryFilter(
    std::unique_ptr<device::BluetoothDiscoveryFilter> discovery_filter,
    const base::Closure& callback,
    DiscoverySessionErrorCallback error_callback) {
  NOTREACHED();
}

void FakeCentral::RemovePairingDelegateInternal(
    device::BluetoothDevice::PairingDelegate* pairing_delegate) {
  NOTREACHED();
}

FakeCentral::~FakeCentral() = default;

FakePeripheral* FakeCentral::GetFakePeripheral(
    const std::string& peripheral_address) const {
  auto device_iter = devices_.find(peripheral_address);
  if (device_iter == devices_.end()) {
    return nullptr;
  }

  // static_cast is safe because the parent class's devices_ is only
  // populated via this FakeCentral, and only with FakePeripherals.
  return static_cast<FakePeripheral*>(device_iter->second.get());
}

FakeRemoteGattService* FakeCentral::GetFakeRemoteGattService(
    const std::string& peripheral_address,
    const std::string& service_id) const {
  FakePeripheral* fake_peripheral = GetFakePeripheral(peripheral_address);
  if (fake_peripheral == nullptr) {
    return nullptr;
  }

  // static_cast is safe because FakePeripheral is only populated with
  // FakeRemoteGattServices.
  return static_cast<FakeRemoteGattService*>(
      fake_peripheral->GetGattService(service_id));
}

FakeRemoteGattCharacteristic* FakeCentral::GetFakeRemoteGattCharacteristic(
    const std::string& peripheral_address,
    const std::string& service_id,
    const std::string& characteristic_id) const {
  FakeRemoteGattService* fake_remote_gatt_service =
      GetFakeRemoteGattService(peripheral_address, service_id);
  if (fake_remote_gatt_service == nullptr) {
    return nullptr;
  }

  // static_cast is safe because FakeRemoteGattService is only populated with
  // FakeRemoteGattCharacteristics.
  return static_cast<FakeRemoteGattCharacteristic*>(
      fake_remote_gatt_service->GetCharacteristic(characteristic_id));
}

FakeRemoteGattDescriptor* FakeCentral::GetFakeRemoteGattDescriptor(
    const std::string& peripheral_address,
    const std::string& service_id,
    const std::string& characteristic_id,
    const std::string& descriptor_id) const {
  FakeRemoteGattCharacteristic* fake_remote_gatt_characteristic =
      GetFakeRemoteGattCharacteristic(peripheral_address, service_id,
                                      characteristic_id);
  if (fake_remote_gatt_characteristic == nullptr) {
    return nullptr;
  }

  // static_cast is safe because FakeRemoteGattCharacteristic is only populated
  // with FakeRemoteGattDescriptors.
  return static_cast<FakeRemoteGattDescriptor*>(
      fake_remote_gatt_characteristic->GetDescriptor(descriptor_id));
}

}  // namespace bluetooth
