// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/test/bluetooth_test_win.h"

#include <windows.devices.bluetooth.h>
#include <wrl/client.h>
#include <wrl/implements.h>

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/containers/circular_deque.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "base/strings/sys_string_conversions.h"
#include "base/test/test_pending_task.h"
#include "base/time/time.h"
#include "base/win/windows_version.h"
#include "device/base/features.h"
#include "device/bluetooth/bluetooth_adapter_win.h"
#include "device/bluetooth/bluetooth_adapter_winrt.h"
#include "device/bluetooth/bluetooth_low_energy_win.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic_win.h"
#include "device/bluetooth/bluetooth_remote_gatt_descriptor_win.h"
#include "device/bluetooth/bluetooth_remote_gatt_service_win.h"
#include "device/bluetooth/test/fake_bluetooth_adapter_winrt.h"
#include "device/bluetooth/test/fake_device_information_winrt.h"

namespace {

using Microsoft::WRL::Make;
using Microsoft::WRL::ComPtr;
using ABI::Windows::Devices::Bluetooth::IBluetoothAdapter;
using ABI::Windows::Devices::Bluetooth::IBluetoothAdapterStatics;
using ABI::Windows::Devices::Enumeration::IDeviceInformation;
using ABI::Windows::Devices::Enumeration::IDeviceInformationStatics;

class TestBluetoothAdapterWinrt : public device::BluetoothAdapterWinrt {
 public:
  TestBluetoothAdapterWinrt(ComPtr<IBluetoothAdapter> adapter,
                            ComPtr<IDeviceInformation> device_information,
                            InitCallback init_cb)
      : adapter_(std::move(adapter)),
        device_information_(std::move(device_information)) {
    Init(std::move(init_cb));
  }

 protected:
  ~TestBluetoothAdapterWinrt() override = default;

  HRESULT GetBluetoothAdapterStaticsActivationFactory(
      IBluetoothAdapterStatics** statics) const override {
    auto adapter_statics =
        Make<device::FakeBluetoothAdapterStaticsWinrt>(adapter_);
    return adapter_statics.CopyTo(statics);
  };

  HRESULT
  GetDeviceInformationStaticsActivationFactory(
      IDeviceInformationStatics** statics) const override {
    auto device_information_statics =
        Make<device::FakeDeviceInformationStaticsWinrt>(device_information_);
    return device_information_statics.CopyTo(statics);
  };

 private:
  ComPtr<IBluetoothAdapter> adapter_;
  ComPtr<IDeviceInformation> device_information_;
};

BLUETOOTH_ADDRESS CanonicalStringToBLUETOOTH_ADDRESS(
    std::string device_address) {
  BLUETOOTH_ADDRESS win_addr;
  unsigned int data[6];
  int result =
      sscanf_s(device_address.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X",
               &data[5], &data[4], &data[3], &data[2], &data[1], &data[0]);
  CHECK_EQ(6, result);
  for (int i = 0; i < 6; i++) {
    win_addr.rgBytes[i] = data[i];
  }
  return win_addr;
}

// The canonical UUID string format is device::BluetoothUUID.value().
BTH_LE_UUID CanonicalStringToBTH_LE_UUID(std::string uuid) {
  BTH_LE_UUID win_uuid = {0};
  if (uuid.size() == 4) {
    win_uuid.IsShortUuid = TRUE;
    unsigned int data[1];
    int result = sscanf_s(uuid.c_str(), "%04x", &data[0]);
    CHECK_EQ(1, result);
    win_uuid.Value.ShortUuid = data[0];
  } else if (uuid.size() == 36) {
    win_uuid.IsShortUuid = FALSE;
    unsigned int data[11];
    int result = sscanf_s(
        uuid.c_str(), "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        &data[0], &data[1], &data[2], &data[3], &data[4], &data[5], &data[6],
        &data[7], &data[8], &data[9], &data[10]);
    CHECK_EQ(11, result);
    win_uuid.Value.LongUuid.Data1 = data[0];
    win_uuid.Value.LongUuid.Data2 = data[1];
    win_uuid.Value.LongUuid.Data3 = data[2];
    win_uuid.Value.LongUuid.Data4[0] = data[3];
    win_uuid.Value.LongUuid.Data4[1] = data[4];
    win_uuid.Value.LongUuid.Data4[2] = data[5];
    win_uuid.Value.LongUuid.Data4[3] = data[6];
    win_uuid.Value.LongUuid.Data4[4] = data[7];
    win_uuid.Value.LongUuid.Data4[5] = data[8];
    win_uuid.Value.LongUuid.Data4[6] = data[9];
    win_uuid.Value.LongUuid.Data4[7] = data[10];
  } else {
    CHECK(false);
  }

  return win_uuid;
}

}  // namespace

namespace device {

BluetoothTestWin::BluetoothTestWin()
    : ui_task_runner_(new base::TestSimpleTaskRunner()),
      bluetooth_task_runner_(new base::TestSimpleTaskRunner()),
      fake_bt_classic_wrapper_(nullptr),
      fake_bt_le_wrapper_(nullptr) {}
BluetoothTestWin::~BluetoothTestWin() {}

bool BluetoothTestWin::PlatformSupportsLowEnergy() {
  if (fake_bt_le_wrapper_)
    return fake_bt_le_wrapper_->IsBluetoothLowEnergySupported();
  return true;
}

void BluetoothTestWin::InitWithDefaultAdapter() {
  if (BluetoothAdapterWin::UseNewBLEWinImplementation()) {
    base::RunLoop run_loop;
    auto adapter = base::WrapRefCounted(new BluetoothAdapterWinrt());
    adapter->Init(run_loop.QuitClosure());
    adapter_ = std::move(adapter);
    run_loop.Run();
    return;
  }

  auto adapter =
      base::WrapRefCounted(new BluetoothAdapterWin(base::DoNothing()));
  adapter->Init();
  adapter_ = std::move(adapter);
}

void BluetoothTestWin::InitWithoutDefaultAdapter() {
  if (BluetoothAdapterWin::UseNewBLEWinImplementation()) {
    base::RunLoop run_loop;
    adapter_ = base::MakeRefCounted<TestBluetoothAdapterWinrt>(
        nullptr, nullptr, run_loop.QuitClosure());
    run_loop.Run();
    return;
  }

  auto adapter =
      base::WrapRefCounted(new BluetoothAdapterWin(base::DoNothing()));
  adapter->InitForTest(ui_task_runner_, bluetooth_task_runner_);
  adapter_ = std::move(adapter);
}

void BluetoothTestWin::InitWithFakeAdapter() {
  if (BluetoothAdapterWin::UseNewBLEWinImplementation()) {
    base::RunLoop run_loop;
    adapter_ = base::MakeRefCounted<TestBluetoothAdapterWinrt>(
        Make<FakeBluetoothAdapterWinrt>(kTestAdapterAddress),
        Make<FakeDeviceInformationWinrt>(kTestAdapterName),
        run_loop.QuitClosure());
    run_loop.Run();
    return;
  }

  fake_bt_classic_wrapper_ = new win::BluetoothClassicWrapperFake();
  fake_bt_le_wrapper_ = new win::BluetoothLowEnergyWrapperFake();
  fake_bt_le_wrapper_->AddObserver(this);
  win::BluetoothClassicWrapper::SetInstanceForTest(fake_bt_classic_wrapper_);
  win::BluetoothLowEnergyWrapper::SetInstanceForTest(fake_bt_le_wrapper_);
  fake_bt_classic_wrapper_->SimulateARadio(
      base::SysUTF8ToWide(kTestAdapterName),
      CanonicalStringToBLUETOOTH_ADDRESS(kTestAdapterAddress));

  auto adapter =
      base::WrapRefCounted(new BluetoothAdapterWin(base::DoNothing()));
  adapter->InitForTest(nullptr, bluetooth_task_runner_);
  adapter_ = std::move(adapter);
  FinishPendingTasks();
}

bool BluetoothTestWin::DenyPermission() {
  return false;
}

void BluetoothTestWin::StartLowEnergyDiscoverySession() {
  __super ::StartLowEnergyDiscoverySession();
  FinishPendingTasks();
}

BluetoothDevice* BluetoothTestWin::SimulateLowEnergyDevice(int device_ordinal) {
  if (device_ordinal > 4 || device_ordinal < 1)
    return nullptr;

  std::string device_name = kTestDeviceName;
  std::string device_address = kTestDeviceAddress1;
  std::string service_uuid_1;
  std::string service_uuid_2;

  switch (device_ordinal) {
    case 1:
      service_uuid_1 = kTestUUIDGenericAccess;
      service_uuid_2 = kTestUUIDGenericAttribute;
      break;
    case 2:
      service_uuid_1 = kTestUUIDImmediateAlert;
      service_uuid_2 = kTestUUIDLinkLoss;
      break;
    case 3:
      device_name = kTestDeviceNameEmpty;
      break;
    case 4:
      device_name = kTestDeviceNameEmpty;
      device_address = kTestDeviceAddress2;
      break;
  }

  win::BLEDevice* simulated_device = fake_bt_le_wrapper_->SimulateBLEDevice(
      device_name, CanonicalStringToBLUETOOTH_ADDRESS(device_address));
  if (simulated_device != nullptr) {
    if (!service_uuid_1.empty()) {
      fake_bt_le_wrapper_->SimulateGattService(
          simulated_device, nullptr,
          CanonicalStringToBTH_LE_UUID(service_uuid_1));
    }
    if (!service_uuid_2.empty()) {
      fake_bt_le_wrapper_->SimulateGattService(
          simulated_device, nullptr,
          CanonicalStringToBTH_LE_UUID(service_uuid_2));
    }
  }
  FinishPendingTasks();

  std::vector<BluetoothDevice*> devices = adapter_->GetDevices();
  for (auto* device : devices) {
    if (device->GetAddress() == device_address)
      return device;
  }

  return nullptr;
}

void BluetoothTestWin::SimulateGattConnection(BluetoothDevice* device) {
  FinishPendingTasks();
  // We don't actually attempt to discover on Windows, so fake it for testing.
  gatt_discovery_attempts_++;
}

void BluetoothTestWin::SimulateGattServicesDiscovered(
    BluetoothDevice* device,
    const std::vector<std::string>& uuids) {
  std::string address =
      device ? device->GetAddress() : remembered_device_address_;

  win::BLEDevice* simulated_device =
      fake_bt_le_wrapper_->GetSimulatedBLEDevice(address);
  CHECK(simulated_device);

  for (auto uuid : uuids) {
    fake_bt_le_wrapper_->SimulateGattService(
        simulated_device, nullptr, CanonicalStringToBTH_LE_UUID(uuid));
  }

  FinishPendingTasks();

  // We still need to discover characteristics.  Wait for the appropriate method
  // to be posted and then finish the pending tasks.
  base::RunLoop().RunUntilIdle();
  FinishPendingTasks();
}

void BluetoothTestWin::SimulateGattServiceRemoved(
    BluetoothRemoteGattService* service) {
  std::string device_address = service->GetDevice()->GetAddress();
  win::BLEDevice* target_device =
      fake_bt_le_wrapper_->GetSimulatedBLEDevice(device_address);
  CHECK(target_device);

  BluetoothRemoteGattServiceWin* win_service =
      static_cast<BluetoothRemoteGattServiceWin*>(service);
  std::string service_att_handle =
      std::to_string(win_service->GetAttributeHandle());
  fake_bt_le_wrapper_->SimulateGattServiceRemoved(target_device, nullptr,
                                                  service_att_handle);

  ForceRefreshDevice();
}

void BluetoothTestWin::SimulateGattCharacteristic(
    BluetoothRemoteGattService* service,
    const std::string& uuid,
    int properties) {
  std::string device_address = service->GetDevice()->GetAddress();
  win::BLEDevice* target_device =
      fake_bt_le_wrapper_->GetSimulatedBLEDevice(device_address);
  CHECK(target_device);
  win::GattService* target_service =
      GetSimulatedService(target_device, service);
  CHECK(target_service);

  BTH_LE_GATT_CHARACTERISTIC win_characteristic_info;
  win_characteristic_info.CharacteristicUuid =
      CanonicalStringToBTH_LE_UUID(uuid);
  win_characteristic_info.IsBroadcastable = FALSE;
  win_characteristic_info.IsReadable = FALSE;
  win_characteristic_info.IsWritableWithoutResponse = FALSE;
  win_characteristic_info.IsWritable = FALSE;
  win_characteristic_info.IsNotifiable = FALSE;
  win_characteristic_info.IsIndicatable = FALSE;
  win_characteristic_info.IsSignedWritable = FALSE;
  win_characteristic_info.HasExtendedProperties = FALSE;
  if (properties & BluetoothRemoteGattCharacteristic::PROPERTY_BROADCAST)
    win_characteristic_info.IsBroadcastable = TRUE;
  if (properties & BluetoothRemoteGattCharacteristic::PROPERTY_READ)
    win_characteristic_info.IsReadable = TRUE;
  if (properties &
      BluetoothRemoteGattCharacteristic::PROPERTY_WRITE_WITHOUT_RESPONSE)
    win_characteristic_info.IsWritableWithoutResponse = TRUE;
  if (properties & BluetoothRemoteGattCharacteristic::PROPERTY_WRITE)
    win_characteristic_info.IsWritable = TRUE;
  if (properties & BluetoothRemoteGattCharacteristic::PROPERTY_NOTIFY)
    win_characteristic_info.IsNotifiable = TRUE;
  if (properties & BluetoothRemoteGattCharacteristic::PROPERTY_INDICATE)
    win_characteristic_info.IsIndicatable = TRUE;
  if (properties &
      BluetoothRemoteGattCharacteristic::PROPERTY_AUTHENTICATED_SIGNED_WRITES) {
    win_characteristic_info.IsSignedWritable = TRUE;
  }
  if (properties &
      BluetoothRemoteGattCharacteristic::PROPERTY_EXTENDED_PROPERTIES)
    win_characteristic_info.HasExtendedProperties = TRUE;

  fake_bt_le_wrapper_->SimulateGattCharacterisc(device_address, target_service,
                                                win_characteristic_info);

  ForceRefreshDevice();
}

void BluetoothTestWin::SimulateGattCharacteristicRemoved(
    BluetoothRemoteGattService* service,
    BluetoothRemoteGattCharacteristic* characteristic) {
  CHECK(service);
  CHECK(characteristic);

  std::string device_address = service->GetDevice()->GetAddress();
  win::GattService* target_service = GetSimulatedService(
      fake_bt_le_wrapper_->GetSimulatedBLEDevice(device_address), service);
  CHECK(target_service);

  std::string characteristic_att_handle = std::to_string(
      static_cast<BluetoothRemoteGattCharacteristicWin*>(characteristic)
          ->GetAttributeHandle());
  fake_bt_le_wrapper_->SimulateGattCharacteriscRemove(
      target_service, characteristic_att_handle);

  ForceRefreshDevice();
}

void BluetoothTestWin::RememberCharacteristicForSubsequentAction(
    BluetoothRemoteGattCharacteristic* characteristic) {
  CHECK(characteristic);
  BluetoothRemoteGattCharacteristicWin* win_characteristic =
      static_cast<BluetoothRemoteGattCharacteristicWin*>(characteristic);

  std::string device_address =
      win_characteristic->GetService()->GetDevice()->GetAddress();
  win::BLEDevice* target_device =
      fake_bt_le_wrapper_->GetSimulatedBLEDevice(device_address);
  CHECK(target_device);
  win::GattService* target_service =
      GetSimulatedService(target_device, win_characteristic->GetService());
  CHECK(target_service);
  fake_bt_le_wrapper_->RememberCharacteristicForSubsequentAction(
      target_service, std::to_string(win_characteristic->GetAttributeHandle()));
}

void BluetoothTestWin::SimulateGattCharacteristicRead(
    BluetoothRemoteGattCharacteristic* characteristic,
    const std::vector<uint8_t>& value) {
  win::GattCharacteristic* target_simulated_characteristic = nullptr;
  if (characteristic) {
    target_simulated_characteristic =
        GetSimulatedCharacteristic(characteristic);
  }

  fake_bt_le_wrapper_->SimulateGattCharacteristicValue(
      target_simulated_characteristic, value);

  RunPendingTasksUntilCallback();
}

void BluetoothTestWin::SimulateGattCharacteristicReadError(
    BluetoothRemoteGattCharacteristic* characteristic,
    BluetoothRemoteGattService::GattErrorCode error_code) {
  win::GattCharacteristic* target_characteristic =
      GetSimulatedCharacteristic(characteristic);
  CHECK(target_characteristic);
  HRESULT hr = HRESULT_FROM_WIN32(ERROR_SEM_TIMEOUT);
  if (error_code == BluetoothRemoteGattService::GATT_ERROR_INVALID_LENGTH)
    hr = E_BLUETOOTH_ATT_INVALID_ATTRIBUTE_VALUE_LENGTH;
  fake_bt_le_wrapper_->SimulateGattCharacteristicReadError(
      target_characteristic, hr);

  FinishPendingTasks();
}

void BluetoothTestWin::SimulateGattCharacteristicWrite(
    BluetoothRemoteGattCharacteristic* characteristic) {
  RunPendingTasksUntilCallback();
}

void BluetoothTestWin::SimulateGattCharacteristicWriteError(
    BluetoothRemoteGattCharacteristic* characteristic,
    BluetoothRemoteGattService::GattErrorCode error_code) {
  win::GattCharacteristic* target_characteristic =
      GetSimulatedCharacteristic(characteristic);
  CHECK(target_characteristic);
  HRESULT hr = HRESULT_FROM_WIN32(ERROR_SEM_TIMEOUT);
  if (error_code == BluetoothRemoteGattService::GATT_ERROR_INVALID_LENGTH)
    hr = E_BLUETOOTH_ATT_INVALID_ATTRIBUTE_VALUE_LENGTH;
  fake_bt_le_wrapper_->SimulateGattCharacteristicWriteError(
      target_characteristic, hr);

  FinishPendingTasks();
}

void BluetoothTestWin::RememberDeviceForSubsequentAction(
    BluetoothDevice* device) {
  remembered_device_address_ = device->GetAddress();
}

void BluetoothTestWin::DeleteDevice(BluetoothDevice* device) {
  CHECK(device);
  fake_bt_le_wrapper_->RemoveSimulatedBLEDevice(device->GetAddress());
  FinishPendingTasks();
}

void BluetoothTestWin::SimulateGattDescriptor(
    BluetoothRemoteGattCharacteristic* characteristic,
    const std::string& uuid) {
  win::GattCharacteristic* target_characteristic =
      GetSimulatedCharacteristic(characteristic);
  CHECK(target_characteristic);
  fake_bt_le_wrapper_->SimulateGattDescriptor(
      characteristic->GetService()->GetDevice()->GetAddress(),
      target_characteristic, CanonicalStringToBTH_LE_UUID(uuid));
  ForceRefreshDevice();
}

void BluetoothTestWin::SimulateGattNotifySessionStarted(
    BluetoothRemoteGattCharacteristic* characteristic) {
  FinishPendingTasks();
}

void BluetoothTestWin::SimulateGattNotifySessionStartError(
    BluetoothRemoteGattCharacteristic* characteristic,
    BluetoothRemoteGattService::GattErrorCode error_code) {
  win::GattCharacteristic* simulated_characteristic =
      GetSimulatedCharacteristic(characteristic);
  DCHECK(simulated_characteristic);
  DCHECK(error_code == BluetoothRemoteGattService::GATT_ERROR_UNKNOWN);
  fake_bt_le_wrapper_->SimulateGattCharacteristicSetNotifyError(
      simulated_characteristic, E_BLUETOOTH_ATT_UNKNOWN_ERROR);
}

void BluetoothTestWin::SimulateGattCharacteristicChanged(
    BluetoothRemoteGattCharacteristic* characteristic,
    const std::vector<uint8_t>& value) {
  win::GattCharacteristic* target_simulated_characteristic = nullptr;
  if (characteristic) {
    target_simulated_characteristic =
        GetSimulatedCharacteristic(characteristic);
  }

  fake_bt_le_wrapper_->SimulateGattCharacteristicValue(
      target_simulated_characteristic, value);
  fake_bt_le_wrapper_->SimulateCharacteristicValueChangeNotification(
      target_simulated_characteristic);

  FinishPendingTasks();
}

void BluetoothTestWin::OnReadGattCharacteristicValue() {
  gatt_read_characteristic_attempts_++;
}

void BluetoothTestWin::OnWriteGattCharacteristicValue(
    const PBTH_LE_GATT_CHARACTERISTIC_VALUE value) {
  gatt_write_characteristic_attempts_++;
  last_write_value_.clear();
  for (ULONG i = 0; i < value->DataSize; i++)
    last_write_value_.push_back(value->Data[i]);
}

void BluetoothTestWin::OnStartCharacteristicNotification() {
  gatt_notify_characteristic_attempts_++;
}

void BluetoothTestWin::OnWriteGattDescriptorValue(
    const std::vector<uint8_t>& value) {
  gatt_write_descriptor_attempts_++;
  last_write_value_.assign(value.begin(), value.end());
}

win::GattService* BluetoothTestWin::GetSimulatedService(
    win::BLEDevice* device,
    BluetoothRemoteGattService* service) {
  CHECK(device);
  CHECK(service);

  std::vector<std::string> chain_of_att_handles;
  BluetoothRemoteGattServiceWin* win_service =
      static_cast<BluetoothRemoteGattServiceWin*>(service);
  chain_of_att_handles.insert(
      chain_of_att_handles.begin(),
      std::to_string(win_service->GetAttributeHandle()));
  win::GattService* simulated_service =
      fake_bt_le_wrapper_->GetSimulatedGattService(device,
                                                   chain_of_att_handles);
  CHECK(simulated_service);
  return simulated_service;
}

win::GattCharacteristic* BluetoothTestWin::GetSimulatedCharacteristic(
    BluetoothRemoteGattCharacteristic* characteristic) {
  CHECK(characteristic);
  BluetoothRemoteGattCharacteristicWin* win_characteristic =
      static_cast<BluetoothRemoteGattCharacteristicWin*>(characteristic);

  std::string device_address =
      win_characteristic->GetService()->GetDevice()->GetAddress();
  win::BLEDevice* target_device =
      fake_bt_le_wrapper_->GetSimulatedBLEDevice(device_address);
  if (target_device == nullptr)
    return nullptr;
  win::GattService* target_service =
      GetSimulatedService(target_device, win_characteristic->GetService());
  if (target_service == nullptr)
    return nullptr;
  return fake_bt_le_wrapper_->GetSimulatedGattCharacteristic(
      target_service, std::to_string(win_characteristic->GetAttributeHandle()));
}

void BluetoothTestWin::RunPendingTasksUntilCallback() {
  base::circular_deque<base::TestPendingTask> tasks =
      bluetooth_task_runner_->TakePendingTasks();
  int original_callback_count = callback_count_;
  int original_error_callback_count = error_callback_count_;
  do {
    base::TestPendingTask task = std::move(tasks.front());
    tasks.pop_front();
    std::move(task.task).Run();
    base::RunLoop().RunUntilIdle();
  } while (tasks.size() && callback_count_ == original_callback_count &&
           error_callback_count_ == original_error_callback_count);

  // Put the rest of pending tasks back to Bluetooth task runner.
  for (auto& task : tasks) {
    if (task.delay.is_zero()) {
      bluetooth_task_runner_->PostTask(task.location, std::move(task.task));
    } else {
      bluetooth_task_runner_->PostDelayedTask(task.location,
                                              std::move(task.task), task.delay);
    }
  }
}

void BluetoothTestWin::ForceRefreshDevice() {
  auto* adapter_win = static_cast<BluetoothAdapterWin*>(adapter_.get());
  adapter_win->force_update_device_for_test_ = true;
  FinishPendingTasks();
  adapter_win->force_update_device_for_test_ = false;

  // The characteristics still need to be discovered.
  base::RunLoop().RunUntilIdle();
  FinishPendingTasks();
}

void BluetoothTestWin::FinishPendingTasks() {
  bluetooth_task_runner_->RunPendingTasks();
  base::RunLoop().RunUntilIdle();
}

BluetoothTestWinrt::BluetoothTestWinrt() {
  if (GetParam()) {
    scoped_feature_list_.InitAndEnableFeature(kNewBLEWinImplementation);
    if (base::win::GetVersion() >= base::win::VERSION_WIN10) {
      scoped_winrt_initializer_.emplace();
    }
  } else {
    scoped_feature_list_.InitAndDisableFeature(kNewBLEWinImplementation);
  }
}

BluetoothTestWinrt::~BluetoothTestWinrt() = default;

}  // namespace device
