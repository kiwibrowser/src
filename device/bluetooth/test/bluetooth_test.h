// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_TEST_BLUETOOTH_TEST_H_
#define DEVICE_BLUETOOTH_TEST_BLUETOOTH_TEST_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/test/scoped_task_environment.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_discovery_session.h"
#include "device/bluetooth/bluetooth_gatt_connection.h"
#include "device/bluetooth/bluetooth_gatt_notify_session.h"
#include "device/bluetooth/bluetooth_local_gatt_service.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_remote_gatt_descriptor.h"
#include "device/bluetooth/bluetooth_remote_gatt_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

class BluetoothAdapter;
class BluetoothDevice;
class BluetoothLocalGattCharacteristic;
class BluetoothLocalGattDescriptor;

// A test fixture for Bluetooth that abstracts platform specifics for creating
// and controlling fake low level objects.
//
// Subclasses on each platform implement this, and are then typedef-ed to
// BluetoothTest.
class BluetoothTestBase : public testing::Test {
 public:
  enum class Call { EXPECTED, NOT_EXPECTED };

  // List of devices that can be simulated with
  // SimulateConnectedLowEnergyDevice().
  // GENERIC_DEVICE:
  //   - Name:     kTestDeviceName
  //   - Address:  kTestPeripheralUUID1
  //   - Services: [ kTestUUIDGenericAccess ]
  // HEART_RATE_DEVICE:
  //   - Name:     kTestDeviceName
  //   - Address:  kTestPeripheralUUID2
  //   - Services: [ kTestUUIDGenericAccess, kTestUUIDHeartRate]
  enum class ConnectedDeviceType {
    GENERIC_DEVICE,
    HEART_RATE_DEVICE,
  };

  enum class NotifyValueState {
    NONE,
    NOTIFY,
    INDICATE,
  };

  static const char kTestAdapterName[];
  static const char kTestAdapterAddress[];

  static const char kTestDeviceName[];
  static const char kTestDeviceNameEmpty[];
  static const char kTestDeviceNameU2f[];

  static const char kTestDeviceAddress1[];
  static const char kTestDeviceAddress2[];
  static const char kTestDeviceAddress3[];

  // GENERATED_JAVA_ENUM_PACKAGE: org.chromium.device.bluetooth.test
  enum class TestRSSI {
    LOWEST = -81,
    LOWER = -61,
    LOW = -41,
    MEDIUM = -21,
    HIGH = -1,
  };

  // GENERATED_JAVA_ENUM_PACKAGE: org.chromium.device.bluetooth.test
  enum class TestTxPower {
    LOWEST = -40,
    LOWER = -20,
  };

  // Services
  static const char kTestUUIDGenericAccess[];
  static const char kTestUUIDGenericAttribute[];
  static const char kTestUUIDImmediateAlert[];
  static const char kTestUUIDLinkLoss[];
  static const char kTestUUIDHeartRate[];
  static const char kTestUUIDU2f[];
  // Characteristics
  // The following three characteristics are for kTestUUIDGenericAccess.
  static const char kTestUUIDDeviceName[];
  static const char kTestUUIDAppearance[];
  static const char kTestUUIDReconnectionAddress[];
  // This characteristic is for kTestUUIDHeartRate.
  static const char kTestUUIDHeartRateMeasurement[];
  // This characteristic is for kTestUUIDU2f.
  static const char kTestUUIDU2fControlPointLength[];
  // Descriptors
  static const char kTestUUIDCharacteristicUserDescription[];
  static const char kTestUUIDClientCharacteristicConfiguration[];
  static const char kTestUUIDServerCharacteristicConfiguration[];
  static const char kTestUUIDCharacteristicPresentationFormat[];
  // Manufacturer data
  static const unsigned short kTestManufacturerId;

  BluetoothTestBase();
  ~BluetoothTestBase() override;

  // Checks that no unexpected calls have been made to callbacks.
  // Overrides of this method should always call the parent's class method.
  void TearDown() override;

  // Calls adapter_->StartDiscoverySessionWithFilter with Low Energy transport,
  // and this fixture's callbacks expecting success.
  // Then RunLoop().RunUntilIdle().
  virtual void StartLowEnergyDiscoverySession();

  // Calls adapter_->StartDiscoverySessionWithFilter with Low Energy transport,
  // and this fixture's callbacks expecting error.
  // Then RunLoop().RunUntilIdle().
  void StartLowEnergyDiscoverySessionExpectedToFail();

  // Check if Low Energy is available. On Mac, we require OS X >= 10.10.
  virtual bool PlatformSupportsLowEnergy() = 0;

  // Initializes the BluetoothAdapter |adapter_| with the system adapter.
  virtual void InitWithDefaultAdapter() {}

  // Initializes the BluetoothAdapter |adapter_| with the system adapter forced
  // to be ignored as if it did not exist. This enables tests for when an
  // adapter is not present on the system.
  virtual void InitWithoutDefaultAdapter() {}

  // Initializes the BluetoothAdapter |adapter_| with a fake adapter that can be
  // controlled by this test fixture.
  virtual void InitWithFakeAdapter() {}

  // Configures the fake adapter to lack the necessary permissions to scan for
  // devices.  Returns false if the current platform always has permission.
  virtual bool DenyPermission();

  // Simulates the Adapter being switched off.
  virtual void SimulateAdapterPoweredOff() {}

  // Create a fake Low Energy device and discover it.
  // |device_ordinal| with the same device address stands for the same fake
  // device with different properties.
  // For example:
  // SimulateLowEnergyDevice(2); << First call will create a device with address
  // kTestDeviceAddress1
  // SimulateLowEnergyDevice(3); << Second call will update changes to the
  // device of address kTestDeviceAddress1.
  //
  // |device_ordinal| selects between multiple fake device data sets to produce:
  //   1: Name: kTestDeviceName
  //      Address:           kTestDeviceAddress1
  //      RSSI:              kTestRSSI1
  //      Advertised UUIDs: {kTestUUIDGenericAccess, kTestUUIDGenericAttribute}
  //      Service Data:     {kTestUUIDHeartRate: [1]}
  //      ManufacturerData: {kTestManufacturerId: [1, 2, 3, 4]}
  //      Tx Power:          kTestTxPower1
  //   2: Name: kTestDeviceName
  //      Address:           kTestDeviceAddress1
  //      RSSI:              kTestRSSI2
  //      Advertised UUIDs: {kTestUUIDImmediateAlert, kTestUUIDLinkLoss}
  //      Service Data:     {kTestUUIDHeartRate: [],
  //                         kTestUUIDImmediateAlert: [0, 2]}
  //      ManufacturerData: {kTestManufacturerId: []}
  //      Tx Power:          kTestTxPower2
  //   3: Name:    kTestDeviceNameEmpty
  //      Address: kTestDeviceAddress1
  //      RSSI:    kTestRSSI3
  //      No Advertised UUIDs
  //      No Service Data
  //      No Manufacturer Data
  //      No Tx Power
  //   4: Name:    kTestDeviceNameEmpty
  //      Address: kTestDeviceAddress2
  //      RSSI:    kTestRSSI4
  //      No Advertised UUIDs
  //      No Service Data
  //      No Manufacturer Data
  //      No Tx Power
  //   5: No name device
  //      Address: kTestDeviceAddress1
  //      RSSI:    kTestRSSI5
  //      No Advertised UUIDs
  //      No Service Data
  //      No Tx Power
  //   6: Name:    kTestDeviceName
  //      Address: kTestDeviceAddress2
  //      RSSI:    kTestRSSI1,
  //      No Advertised UUIDs
  //      No Service Data
  //      No Manufacturer Data
  //      No Tx Power
  //      Supports BR/EDR and LE.
  //   7: Name:    kTestDeviceNameU2f
  //      Address: kTestDeviceAddress1
  //      RSSI:    kTestRSSI1,
  //      Advertised UUIDs: {kTestUUIDU2fControlPointLength}
  //      Service Data:     {kTestUUIDU2fControlPointLength: [0, 20]}
  //      No Manufacturer Data
  //      No Tx Power
  //      Supports LE.
  virtual BluetoothDevice* SimulateLowEnergyDevice(int device_ordinal);

  // Simulates a connected low energy device. Used before starting a low energy
  // discovey session.
  virtual void SimulateConnectedLowEnergyDevice(
      ConnectedDeviceType device_ordinal){};

  // Create a fake classic device and discover it. The device will have
  // name kTestDeviceName, no advertised UUIDs and address kTestDeviceAddress3.
  virtual BluetoothDevice* SimulateClassicDevice();

  // Remembers |device|'s platform specific object to be used in a
  // subsequent call to methods such as SimulateGattServicesDiscovered that
  // accept a nullptr value to select this remembered characteristic. This
  // enables tests where the platform attempts to reference device
  // objects after the Chrome objects have been deleted, e.g. with DeleteDevice.
  virtual void RememberDeviceForSubsequentAction(BluetoothDevice* device) {}

  // Simulates success of implementation details of CreateGattConnection.
  virtual void SimulateGattConnection(BluetoothDevice* device) {}

  // Simulates failure of CreateGattConnection with the given error code.
  virtual void SimulateGattConnectionError(BluetoothDevice* device,
                                           BluetoothDevice::ConnectErrorCode) {}

  // Simulates GattConnection disconnecting.
  virtual void SimulateGattDisconnection(BluetoothDevice* device) {}

  // Simulates success of discovering services. |uuids| is used to create a
  // service for each UUID string. Multiple UUIDs with the same value produce
  // multiple service instances.
  virtual void SimulateGattServicesDiscovered(
      BluetoothDevice* device,
      const std::vector<std::string>& uuids) {}

  // Simulates a GATT Services changed event.
  virtual void SimulateGattServicesChanged(BluetoothDevice* device) {}

  // Simulates remove of a |service|.
  virtual void SimulateGattServiceRemoved(BluetoothRemoteGattService* service) {
  }

  // Simulates failure to discover services.
  virtual void SimulateGattServicesDiscoveryError(BluetoothDevice* device) {}

  // Simulates a Characteristic on a service.
  virtual void SimulateGattCharacteristic(BluetoothRemoteGattService* service,
                                          const std::string& uuid,
                                          int properties) {}

  // Simulates remove of a |characteristic| from |service|.
  virtual void SimulateGattCharacteristicRemoved(
      BluetoothRemoteGattService* service,
      BluetoothRemoteGattCharacteristic* characteristic) {}

  // Remembers |characteristic|'s platform specific object to be used in a
  // subsequent call to methods such as SimulateGattCharacteristicRead that
  // accept a nullptr value to select this remembered characteristic. This
  // enables tests where the platform attempts to reference characteristic
  // objects after the Chrome objects have been deleted, e.g. with DeleteDevice.
  virtual void RememberCharacteristicForSubsequentAction(
      BluetoothRemoteGattCharacteristic* characteristic) {}

  // Remembers |characteristic|'s Client Characteristic Configuration (CCC)
  // descriptor's platform specific object to be used in a subsequent call to
  // methods such as SimulateGattNotifySessionStarted. This enables tests where
  // the platform attempts to reference descriptor objects after the Chrome
  // objects have been deleted, e.g. with DeleteDevice.
  virtual void RememberCCCDescriptorForSubsequentAction(
      BluetoothRemoteGattCharacteristic* characteristic) {}

  // Simulates a Characteristic Set Notify success.
  // If |characteristic| is null, acts upon the characteristic & CCC
  // descriptor provided to RememberCharacteristicForSubsequentAction &
  // RememberCCCDescriptorForSubsequentAction.
  virtual void SimulateGattNotifySessionStarted(
      BluetoothRemoteGattCharacteristic* characteristic) {}

  // Simulates a Characteristic Set Notify error.
  // If |characteristic| is null, acts upon the characteristic & CCC
  // descriptor provided to RememberCharacteristicForSubsequentAction &
  // RememberCCCDescriptorForSubsequentAction.
  virtual void SimulateGattNotifySessionStartError(
      BluetoothRemoteGattCharacteristic* characteristic,
      BluetoothRemoteGattService::GattErrorCode error_code) {}

  // Simulates a Characteristic Stop Notify completed.
  // If |characteristic| is null, acts upon the characteristic & CCC
  // descriptor provided to RememberCharacteristicForSubsequentAction &
  // RememberCCCDescriptorForSubsequentAction.
  virtual void SimulateGattNotifySessionStopped(
      BluetoothRemoteGattCharacteristic* characteristic) {}

  // Simulates a Characteristic Stop Notify error.
  // If |characteristic| is null, acts upon the characteristic & CCC
  // descriptor provided to RememberCharacteristicForSubsequentAction &
  // RememberCCCDescriptorForSubsequentAction.
  virtual void SimulateGattNotifySessionStopError(
      BluetoothRemoteGattCharacteristic* characteristic,
      BluetoothRemoteGattService::GattErrorCode error_code) {}

  // Simulates a Characteristic Set Notify operation failing synchronously once
  // for an unknown reason.
  virtual void SimulateGattCharacteristicSetNotifyWillFailSynchronouslyOnce(
      BluetoothRemoteGattCharacteristic* characteristic) {}

  // Simulates a Characteristic Changed operation with updated |value|.
  virtual void SimulateGattCharacteristicChanged(
      BluetoothRemoteGattCharacteristic* characteristic,
      const std::vector<uint8_t>& value) {}

  // Simulates a Characteristic Read operation succeeding, returning |value|.
  // If |characteristic| is null, acts upon the characteristic provided to
  // RememberCharacteristicForSubsequentAction.
  virtual void SimulateGattCharacteristicRead(
      BluetoothRemoteGattCharacteristic* characteristic,
      const std::vector<uint8_t>& value) {}

  // Simulates a Characteristic Read operation failing with a GattErrorCode.
  virtual void SimulateGattCharacteristicReadError(
      BluetoothRemoteGattCharacteristic* characteristic,
      BluetoothRemoteGattService::GattErrorCode) {}

  // Simulates a Characteristic Read operation failing synchronously once for an
  // unknown reason.
  virtual void SimulateGattCharacteristicReadWillFailSynchronouslyOnce(
      BluetoothRemoteGattCharacteristic* characteristic) {}

  // Simulates a Characteristic Write operation succeeding, returning |value|.
  // If |characteristic| is null, acts upon the characteristic provided to
  // RememberCharacteristicForSubsequentAction.
  virtual void SimulateGattCharacteristicWrite(
      BluetoothRemoteGattCharacteristic* characteristic) {}

  // Simulates a Characteristic Write operation failing with a GattErrorCode.
  virtual void SimulateGattCharacteristicWriteError(
      BluetoothRemoteGattCharacteristic* characteristic,
      BluetoothRemoteGattService::GattErrorCode) {}

  // Simulates a Characteristic Write operation failing synchronously once for
  // an unknown reason.
  virtual void SimulateGattCharacteristicWriteWillFailSynchronouslyOnce(
      BluetoothRemoteGattCharacteristic* characteristic) {}

  // Simulates a Descriptor on a service.
  virtual void SimulateGattDescriptor(
      BluetoothRemoteGattCharacteristic* characteristic,
      const std::string& uuid) {}

  // Simulates reading a value from a locally hosted GATT characteristic by a
  // remote central device. Returns the value that was read from the local
  // GATT characteristic in the value callback.
  virtual void SimulateLocalGattCharacteristicValueReadRequest(
      BluetoothDevice* from_device,
      BluetoothLocalGattCharacteristic* characteristic,
      const BluetoothLocalGattService::Delegate::ValueCallback& value_callback,
      const base::Closure& error_callback) {}

  // Simulates write a value to a locally hosted GATT characteristic by a
  // remote central device.
  virtual void SimulateLocalGattCharacteristicValueWriteRequest(
      BluetoothDevice* from_device,
      BluetoothLocalGattCharacteristic* characteristic,
      const std::vector<uint8_t>& value_to_write,
      const base::Closure& success_callback,
      const base::Closure& error_callback) {}

  // Simulates reading a value from a locally hosted GATT descriptor by a
  // remote central device. Returns the value that was read from the local
  // GATT descriptor in the value callback.
  virtual void SimulateLocalGattDescriptorValueReadRequest(
      BluetoothDevice* from_device,
      BluetoothLocalGattDescriptor* descriptor,
      const BluetoothLocalGattService::Delegate::ValueCallback& value_callback,
      const base::Closure& error_callback) {}

  // Simulates write a value to a locally hosted GATT descriptor by a
  // remote central device.
  virtual void SimulateLocalGattDescriptorValueWriteRequest(
      BluetoothDevice* from_device,
      BluetoothLocalGattDescriptor* descriptor,
      const std::vector<uint8_t>& value_to_write,
      const base::Closure& success_callback,
      const base::Closure& error_callback) {}

  // Simulates starting or stopping a notification session for a locally
  // hosted GATT characteristic by a remote device. Returns false if we were
  // not able to start or stop notifications.
  virtual bool SimulateLocalGattCharacteristicNotificationsRequest(
      BluetoothLocalGattCharacteristic* characteristic,
      bool start);

  // Returns the value for the last notification that was sent on this
  // characteristic.
  virtual std::vector<uint8_t> LastNotifactionValueForCharacteristic(
      BluetoothLocalGattCharacteristic* characteristic);

  // Remembers |descriptor|'s platform specific object to be used in a
  // subsequent call to methods such as SimulateGattDescriptorRead that
  // accept a nullptr value to select this remembered descriptor. This
  // enables tests where the platform attempts to reference descriptor
  // objects after the Chrome objects have been deleted, e.g. with DeleteDevice.
  virtual void RememberDescriptorForSubsequentAction(
      BluetoothRemoteGattDescriptor* descriptor) {}

  // Simulates a Descriptor Read operation succeeding, returning |value|.
  // If |descriptor| is null, acts upon the descriptor provided to
  // RememberDescriptorForSubsequentAction.
  virtual void SimulateGattDescriptorRead(
      BluetoothRemoteGattDescriptor* descriptor,
      const std::vector<uint8_t>& value) {}

  // Simulates a Descriptor Read operation failing with a GattErrorCode.
  virtual void SimulateGattDescriptorReadError(
      BluetoothRemoteGattDescriptor* descriptor,
      BluetoothRemoteGattService::GattErrorCode) {}

  // Simulates a Descriptor Read operation failing synchronously once for an
  // unknown reason.
  virtual void SimulateGattDescriptorReadWillFailSynchronouslyOnce(
      BluetoothRemoteGattDescriptor* descriptor) {}

  // Simulates a Descriptor Write operation succeeding, returning |value|.
  // If |descriptor| is null, acts upon the descriptor provided to
  // RememberDescriptorForSubsequentAction.
  virtual void SimulateGattDescriptorWrite(
      BluetoothRemoteGattDescriptor* descriptor) {}

  // Simulates a Descriptor Write operation failing with a GattErrorCode.
  virtual void SimulateGattDescriptorWriteError(
      BluetoothRemoteGattDescriptor* descriptor,
      BluetoothRemoteGattService::GattErrorCode) {}

  // Simulates a Descriptor Write operation failing synchronously once for
  // an unknown reason.
  virtual void SimulateGattDescriptorWriteWillFailSynchronouslyOnce(
      BluetoothRemoteGattDescriptor* descriptor) {}

  // Tests that functions to change the notify value have been called |attempts|
  // times.
  virtual void ExpectedChangeNotifyValueAttempts(int attempts);

  // Tests that the notify value is |expected_value_state|. The default
  // implementation checks that the correct value has been written to the CCC
  // Descriptor.
  virtual void ExpectedNotifyValue(NotifyValueState expected_value_state);

  // Returns a list of local GATT services registered with the adapter.
  virtual std::vector<BluetoothLocalGattService*> RegisteredGattServices();

  // Removes the device from the adapter and deletes it.
  virtual void DeleteDevice(BluetoothDevice* device);

  // Callbacks that increment |callback_count_|, |error_callback_count_|:
  void Callback(Call expected);
  void DiscoverySessionCallback(Call expected,
                                std::unique_ptr<BluetoothDiscoverySession>);
  void GattConnectionCallback(Call expected,
                              std::unique_ptr<BluetoothGattConnection>);
  void NotifyCallback(Call expected,
                      std::unique_ptr<BluetoothGattNotifySession>);
  void NotifyCheckForPrecedingCalls(
      int num_of_preceding_calls,
      std::unique_ptr<BluetoothGattNotifySession>);
  void StopNotifyCallback(Call expected);
  void StopNotifyCheckForPrecedingCalls(int num_of_preceding_calls);
  void ReadValueCallback(Call expected, const std::vector<uint8_t>& value);
  void ErrorCallback(Call expected);
  void ConnectErrorCallback(Call expected,
                            enum BluetoothDevice::ConnectErrorCode);
  void GattErrorCallback(Call expected,
                         BluetoothRemoteGattService::GattErrorCode);
  void ReentrantStartNotifySessionSuccessCallback(
      Call expected,
      BluetoothRemoteGattCharacteristic* characteristic,
      std::unique_ptr<BluetoothGattNotifySession> notify_session);
  void ReentrantStartNotifySessionErrorCallback(
      Call expected,
      BluetoothRemoteGattCharacteristic* characteristic,
      bool error_in_reentrant,
      BluetoothGattService::GattErrorCode error_code);

  // Accessors to get callbacks bound to this fixture:
  base::Closure GetCallback(Call expected);
  BluetoothAdapter::DiscoverySessionCallback GetDiscoverySessionCallback(
      Call expected);
  BluetoothDevice::GattConnectionCallback GetGattConnectionCallback(
      Call expected);
  BluetoothRemoteGattCharacteristic::NotifySessionCallback GetNotifyCallback(
      Call expected);
  BluetoothRemoteGattCharacteristic::NotifySessionCallback
  GetNotifyCheckForPrecedingCalls(int num_of_preceding_calls);
  base::Closure GetStopNotifyCallback(Call expected);
  base::Closure GetStopNotifyCheckForPrecedingCalls(int num_of_preceding_calls);
  BluetoothRemoteGattCharacteristic::ValueCallback GetReadValueCallback(
      Call expected);
  BluetoothAdapter::ErrorCallback GetErrorCallback(Call expected);
  BluetoothDevice::ConnectErrorCallback GetConnectErrorCallback(Call expected);
  base::Callback<void(BluetoothRemoteGattService::GattErrorCode)>
  GetGattErrorCallback(Call expected);
  BluetoothRemoteGattCharacteristic::NotifySessionCallback
  GetReentrantStartNotifySessionSuccessCallback(
      Call expected,
      BluetoothRemoteGattCharacteristic* characteristic);
  base::Callback<void(BluetoothGattService::GattErrorCode)>
  GetReentrantStartNotifySessionErrorCallback(
      Call expected,
      BluetoothRemoteGattCharacteristic* characteristic,
      bool error_in_reentrant);

  // Reset all event count members to 0.
  virtual void ResetEventCounts();

  void RemoveTimedOutDevices();

  // A ScopedTaskEnvironment is required by some implementations that will
  // PostTasks and by base::RunLoop().RunUntilIdle() use in this fixture.
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  scoped_refptr<BluetoothAdapter> adapter_;
  std::vector<std::unique_ptr<BluetoothDiscoverySession>> discovery_sessions_;
  std::vector<std::unique_ptr<BluetoothGattConnection>> gatt_connections_;
  enum BluetoothDevice::ConnectErrorCode last_connect_error_code_ =
      BluetoothDevice::ERROR_UNKNOWN;
  std::vector<std::unique_ptr<BluetoothGattNotifySession>> notify_sessions_;
  std::vector<uint8_t> last_read_value_;
  std::vector<uint8_t> last_write_value_;
  BluetoothRemoteGattService::GattErrorCode last_gatt_error_code_;

  int callback_count_ = 0;
  int error_callback_count_ = 0;
  int gatt_connection_attempts_ = 0;
  int gatt_disconnection_attempts_ = 0;
  int gatt_discovery_attempts_ = 0;
  int gatt_notify_characteristic_attempts_ = 0;
  int gatt_read_characteristic_attempts_ = 0;
  int gatt_write_characteristic_attempts_ = 0;
  int gatt_read_descriptor_attempts_ = 0;
  int gatt_write_descriptor_attempts_ = 0;

  // The following values are used to make sure the correct callbacks
  // have been called. They are not reset when calling ResetEventCounts().
  int expected_success_callback_calls_ = 0;
  int expected_error_callback_calls_ = 0;
  int actual_success_callback_calls_ = 0;
  int actual_error_callback_calls_ = 0;
  bool unexpected_success_callback_ = false;
  bool unexpected_error_callback_ = false;

  base::WeakPtrFactory<BluetoothTestBase> weak_factory_;
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_TEST_BLUETOOTH_TEST_H_
