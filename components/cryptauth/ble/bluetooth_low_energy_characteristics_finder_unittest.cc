// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/ble/bluetooth_low_energy_characteristics_finder.h"

#include <memory>

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "components/cryptauth/ble/remote_attribute.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "device/bluetooth/test/mock_bluetooth_adapter.h"
#include "device/bluetooth/test/mock_bluetooth_device.h"
#include "device/bluetooth/test/mock_bluetooth_gatt_characteristic.h"
#include "device/bluetooth/test/mock_bluetooth_gatt_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::AtLeast;
using testing::NiceMock;
using testing::Return;
using testing::StrictMock;
using testing::SaveArg;

namespace cryptauth {
namespace {

const char kDeviceName[] = "Device name";
const char kBluetoothAddress[] = "11:22:33:44:55:66";

const char kServiceUUID[] = "DEADBEEF-CAFE-FEED-FOOD-D15EA5EBEEEF";
const char kToPeripheralCharUUID[] = "FBAE09F2-0482-11E5-8418-1697F925EC7B";
const char kFromPeripheralCharUUID[] = "5539ED10-0483-11E5-8418-1697F925EC7B";

const char kToPeripheralCharID[] = "to peripheral id";
const char kFromPeripheralCharID[] = "from peripheral id";

const device::BluetoothRemoteGattCharacteristic::Properties
    kCharacteristicProperties =
        device::BluetoothRemoteGattCharacteristic::PROPERTY_BROADCAST |
        device::BluetoothRemoteGattCharacteristic::PROPERTY_READ |
        device::BluetoothRemoteGattCharacteristic::
            PROPERTY_WRITE_WITHOUT_RESPONSE |
        device::BluetoothRemoteGattCharacteristic::PROPERTY_INDICATE;

const char kOtherCharUUID[] = "09731422-048A-11E5-8418-1697F925EC7B";
const char kOtherCharID[] = "other id";
}  //  namespace

class CryptAuthBluetoothLowEnergyCharacteristicFinderTest
    : public testing::Test {
 protected:
  CryptAuthBluetoothLowEnergyCharacteristicFinderTest()
      : adapter_(new NiceMock<device::MockBluetoothAdapter>),
        success_callback_(base::Bind(
            &CryptAuthBluetoothLowEnergyCharacteristicFinderTest::
                OnCharacteristicsFound,
            base::Unretained(this))),
        error_callback_(base::Bind(
            &CryptAuthBluetoothLowEnergyCharacteristicFinderTest::
                OnCharacteristicsFinderError,
            base::Unretained(this))),
        device_(new NiceMock<device::MockBluetoothDevice>(adapter_.get(),
                                                          0,
                                                          kDeviceName,
                                                          kBluetoothAddress,
                                                          false,
                                                          false)),
        service_(new NiceMock<device::MockBluetoothGattService>(
            device_.get(),
            "",
            device::BluetoothUUID(kServiceUUID),
            true,
            false)),
        remote_service_({device::BluetoothUUID(kServiceUUID), ""}),
        to_peripheral_char_({device::BluetoothUUID(kToPeripheralCharUUID), ""}),
        from_peripheral_char_(
            {device::BluetoothUUID(kFromPeripheralCharUUID), ""}) {
    device::BluetoothAdapterFactory::SetAdapterForTesting(adapter_);

    // The default behavior for |device_| is to have no services discovered. Can
    // be overrided later.
    ON_CALL(*device_, GetGattServices())
        .WillByDefault(
            Return(std::vector<device::BluetoothRemoteGattService*>()));
  }

  void SetUp() {
    EXPECT_CALL(*adapter_, AddObserver(_));
    EXPECT_CALL(*adapter_, RemoveObserver(_));
  }

  MOCK_METHOD3(OnCharacteristicsFound,
               void(const RemoteAttribute&,
                    const RemoteAttribute&,
                    const RemoteAttribute&));
  MOCK_METHOD2(OnCharacteristicsFinderError,
               void(const RemoteAttribute&, const RemoteAttribute&));

  std::unique_ptr<device::MockBluetoothGattCharacteristic>
  ExpectToFindCharacteristic(const device::BluetoothUUID& uuid,
                             const std::string& id,
                             bool valid) {
    std::unique_ptr<device::MockBluetoothGattCharacteristic> characteristic(
        new NiceMock<device::MockBluetoothGattCharacteristic>(
            service_.get(), id, uuid, true, kCharacteristicProperties,
            device::BluetoothRemoteGattCharacteristic::PERMISSION_NONE));

    ON_CALL(*characteristic.get(), GetUUID()).WillByDefault(Return(uuid));
    if (valid)
      ON_CALL(*characteristic.get(), GetIdentifier()).WillByDefault(Return(id));
    ON_CALL(*characteristic.get(), GetService())
        .WillByDefault(Return(service_.get()));
    return characteristic;
  }

  scoped_refptr<device::MockBluetoothAdapter> adapter_;
  BluetoothLowEnergyCharacteristicsFinder::SuccessCallback success_callback_;
  BluetoothLowEnergyCharacteristicsFinder::ErrorCallback error_callback_;
  std::unique_ptr<device::MockBluetoothDevice> device_;
  std::unique_ptr<device::MockBluetoothGattService> service_;
  RemoteAttribute remote_service_;
  RemoteAttribute to_peripheral_char_;
  RemoteAttribute from_peripheral_char_;
};

TEST_F(CryptAuthBluetoothLowEnergyCharacteristicFinderTest,
       ConstructAndDestroyDontCrash) {
  BluetoothLowEnergyCharacteristicsFinder characteristic_finder(
      adapter_, device_.get(), remote_service_, to_peripheral_char_,
      from_peripheral_char_, success_callback_, error_callback_);
}

TEST_F(CryptAuthBluetoothLowEnergyCharacteristicFinderTest,
       FindRightCharacteristics) {
  BluetoothLowEnergyCharacteristicsFinder characteristic_finder(
      adapter_, device_.get(), remote_service_, to_peripheral_char_,
      from_peripheral_char_, success_callback_, error_callback_);
  // Upcasting |characteristic_finder| to access the virtual protected methods
  // from Observer: GattCharacteristicAdded() and
  // GattDiscoveryCompleteForService().
  device::BluetoothAdapter::Observer* observer =
      static_cast<device::BluetoothAdapter::Observer*>(&characteristic_finder);

  RemoteAttribute found_to_char, found_from_char;
  EXPECT_CALL(*this, OnCharacteristicsFound(_, _, _))
      .WillOnce(
          DoAll(SaveArg<1>(&found_to_char), SaveArg<2>(&found_from_char)));
  EXPECT_CALL(*this, OnCharacteristicsFinderError(_, _)).Times(0);

  std::unique_ptr<device::MockBluetoothGattCharacteristic> from_char =
      ExpectToFindCharacteristic(device::BluetoothUUID(kFromPeripheralCharUUID),
                                 kFromPeripheralCharID, true);
  observer->GattCharacteristicAdded(adapter_.get(), from_char.get());

  std::unique_ptr<device::MockBluetoothGattCharacteristic> to_char =
      ExpectToFindCharacteristic(device::BluetoothUUID(kToPeripheralCharUUID),
                                 kToPeripheralCharID, true);
  observer->GattCharacteristicAdded(adapter_.get(), to_char.get());

  EXPECT_EQ(kToPeripheralCharID, found_to_char.id);
  EXPECT_EQ(kFromPeripheralCharID, found_from_char.id);

  EXPECT_CALL(*service_, GetUUID())
      .WillOnce(Return(device::BluetoothUUID(kServiceUUID)));
  observer->GattDiscoveryCompleteForService(adapter_.get(), service_.get());
}

// Tests that CharacteristicFinder ignores events for other devices.
TEST_F(CryptAuthBluetoothLowEnergyCharacteristicFinderTest,
       FindRightCharacteristicsWrongDevice) {
  // Make CharacteristicFinder which is supposed to listen for other device.
  std::unique_ptr<device::BluetoothDevice> device(
      new NiceMock<device::MockBluetoothDevice>(
          adapter_.get(), 0, kDeviceName, kBluetoothAddress, false, false));
  BluetoothLowEnergyCharacteristicsFinder characteristic_finder(
      adapter_, device.get(), remote_service_, to_peripheral_char_,
      from_peripheral_char_, success_callback_, error_callback_);
  // Upcasting |characteristic_finder| to access the virtual protected methods
  // from Observer: GattCharacteristicAdded() and
  // GattDiscoveryCompleteForService().
  device::BluetoothAdapter::Observer* observer =
      static_cast<device::BluetoothAdapter::Observer*>(&characteristic_finder);

  RemoteAttribute found_to_char, found_from_char;
  // These shouldn't be called at all since the GATT events below are for other
  // devices.
  EXPECT_CALL(*this, OnCharacteristicsFound(_, _, _)).Times(0);
  EXPECT_CALL(*this, OnCharacteristicsFinderError(_, _)).Times(0);

  std::unique_ptr<device::MockBluetoothGattCharacteristic> from_char =
      ExpectToFindCharacteristic(device::BluetoothUUID(kFromPeripheralCharUUID),
                                 kFromPeripheralCharID, true);
  observer->GattCharacteristicAdded(adapter_.get(), from_char.get());

  std::unique_ptr<device::MockBluetoothGattCharacteristic> to_char =
      ExpectToFindCharacteristic(device::BluetoothUUID(kToPeripheralCharUUID),
                                 kToPeripheralCharID, true);
  observer->GattCharacteristicAdded(adapter_.get(), to_char.get());

  EXPECT_CALL(*service_, GetUUID())
      .WillOnce(Return(device::BluetoothUUID(kServiceUUID)));
  observer->GattDiscoveryCompleteForService(adapter_.get(), service_.get());
}

TEST_F(CryptAuthBluetoothLowEnergyCharacteristicFinderTest,
       DidntFindRightCharacteristics) {
  BluetoothLowEnergyCharacteristicsFinder characteristic_finder(
      adapter_, device_.get(), remote_service_, to_peripheral_char_,
      from_peripheral_char_, success_callback_, error_callback_);
  device::BluetoothAdapter::Observer* observer =
      static_cast<device::BluetoothAdapter::Observer*>(&characteristic_finder);

  EXPECT_CALL(*this, OnCharacteristicsFound(_, _, _)).Times(0);
  EXPECT_CALL(*this, OnCharacteristicsFinderError(_, _));

  std::unique_ptr<device::MockBluetoothGattCharacteristic> other_char =
      ExpectToFindCharacteristic(device::BluetoothUUID(kOtherCharUUID),
                                 kOtherCharID, false);
  observer->GattCharacteristicAdded(adapter_.get(), other_char.get());

  EXPECT_CALL(*service_, GetUUID())
      .WillOnce(Return(device::BluetoothUUID(kServiceUUID)));
  observer->GattDiscoveryCompleteForService(adapter_.get(), service_.get());
}

TEST_F(CryptAuthBluetoothLowEnergyCharacteristicFinderTest,
       DidntFindRightCharacteristicsNorService) {
  BluetoothLowEnergyCharacteristicsFinder characteristic_finder(
      adapter_, device_.get(), remote_service_, to_peripheral_char_,
      from_peripheral_char_, success_callback_, error_callback_);
  device::BluetoothAdapter::Observer* observer =
      static_cast<device::BluetoothAdapter::Observer*>(&characteristic_finder);

  EXPECT_CALL(*this, OnCharacteristicsFound(_, _, _)).Times(0);
  EXPECT_CALL(*this, OnCharacteristicsFinderError(_, _));

  std::unique_ptr<device::MockBluetoothGattCharacteristic> other_char =
      ExpectToFindCharacteristic(device::BluetoothUUID(kOtherCharUUID),
                                 kOtherCharID, false);
  observer->GattCharacteristicAdded(adapter_.get(), other_char.get());

  // GattServicesDiscovered event is fired but the service that contains the
  // characteristics has not been found. OnCharacteristicsFinderError is
  // expected to be called.
  observer->GattServicesDiscovered(adapter_.get(), device_.get());
}

TEST_F(CryptAuthBluetoothLowEnergyCharacteristicFinderTest,
       FindOnlyOneRightCharacteristic) {
  BluetoothLowEnergyCharacteristicsFinder characteristic_finder(
      adapter_, device_.get(), remote_service_, to_peripheral_char_,
      from_peripheral_char_, success_callback_, error_callback_);
  device::BluetoothAdapter::Observer* observer =
      static_cast<device::BluetoothAdapter::Observer*>(&characteristic_finder);

  RemoteAttribute found_to_char, found_from_char;
  EXPECT_CALL(*this, OnCharacteristicsFound(_, _, _)).Times(0);
  EXPECT_CALL(*this, OnCharacteristicsFinderError(_, _))
      .WillOnce(
          DoAll(SaveArg<0>(&found_to_char), SaveArg<1>(&found_from_char)));

  std::unique_ptr<device::MockBluetoothGattCharacteristic> from_char =
      ExpectToFindCharacteristic(device::BluetoothUUID(kFromPeripheralCharUUID),
                                 kFromPeripheralCharID, true);
  observer->GattCharacteristicAdded(adapter_.get(), from_char.get());

  EXPECT_CALL(*service_, GetUUID())
      .WillOnce(Return(device::BluetoothUUID(kServiceUUID)));
  observer->GattDiscoveryCompleteForService(adapter_.get(), service_.get());
  EXPECT_EQ(kFromPeripheralCharID, found_from_char.id);
}

TEST_F(CryptAuthBluetoothLowEnergyCharacteristicFinderTest,
       FindWrongCharacteristic_FindRightCharacteristics) {
  BluetoothLowEnergyCharacteristicsFinder characteristic_finder(
      adapter_, device_.get(), remote_service_, to_peripheral_char_,
      from_peripheral_char_, success_callback_, error_callback_);
  device::BluetoothAdapter::Observer* observer =
      static_cast<device::BluetoothAdapter::Observer*>(&characteristic_finder);

  RemoteAttribute found_to_char, found_from_char;
  EXPECT_CALL(*this, OnCharacteristicsFound(_, _, _))
      .WillOnce(
          DoAll(SaveArg<1>(&found_to_char), SaveArg<2>(&found_from_char)));
  EXPECT_CALL(*this, OnCharacteristicsFinderError(_, _)).Times(0);

  std::unique_ptr<device::MockBluetoothGattCharacteristic> other_char =
      ExpectToFindCharacteristic(device::BluetoothUUID(kOtherCharUUID),
                                 kOtherCharID, false);
  observer->GattCharacteristicAdded(adapter_.get(), other_char.get());

  std::unique_ptr<device::MockBluetoothGattCharacteristic> from_char =
      ExpectToFindCharacteristic(device::BluetoothUUID(kFromPeripheralCharUUID),
                                 kFromPeripheralCharID, true);
  observer->GattCharacteristicAdded(adapter_.get(), from_char.get());

  std::unique_ptr<device::MockBluetoothGattCharacteristic> to_char =
      ExpectToFindCharacteristic(device::BluetoothUUID(kToPeripheralCharUUID),
                                 kToPeripheralCharID, true);
  observer->GattCharacteristicAdded(adapter_.get(), to_char.get());

  EXPECT_EQ(kToPeripheralCharID, found_to_char.id);
  EXPECT_EQ(kFromPeripheralCharID, found_from_char.id);

  EXPECT_CALL(*service_, GetUUID())
      .WillOnce(Return(device::BluetoothUUID(kServiceUUID)));
  observer->GattDiscoveryCompleteForService(adapter_.get(), service_.get());
}

TEST_F(CryptAuthBluetoothLowEnergyCharacteristicFinderTest,
       RightCharacteristicsAlreadyPresent) {
  RemoteAttribute found_to_char, found_from_char;
  EXPECT_CALL(*this, OnCharacteristicsFound(_, _, _))
      .WillOnce(
          DoAll(SaveArg<1>(&found_to_char), SaveArg<2>(&found_from_char)));
  EXPECT_CALL(*this, OnCharacteristicsFinderError(_, _)).Times(0);

  std::unique_ptr<device::MockBluetoothGattCharacteristic> from_char =
      ExpectToFindCharacteristic(device::BluetoothUUID(kFromPeripheralCharUUID),
                                 kFromPeripheralCharID, true);

  std::unique_ptr<device::MockBluetoothGattCharacteristic> to_char =
      ExpectToFindCharacteristic(device::BluetoothUUID(kToPeripheralCharUUID),
                                 kToPeripheralCharID, true);

  std::vector<device::BluetoothRemoteGattService*> services;
  services.push_back(service_.get());
  ON_CALL(*device_, GetGattServices()).WillByDefault(Return(services));

  std::vector<device::BluetoothRemoteGattCharacteristic*> characteristics;
  characteristics.push_back(from_char.get());
  characteristics.push_back(to_char.get());
  ON_CALL(*service_, GetCharacteristics())
      .WillByDefault(Return(characteristics));

  BluetoothLowEnergyCharacteristicsFinder characteristic_finder(
      adapter_, device_.get(), remote_service_, to_peripheral_char_,
      from_peripheral_char_, success_callback_, error_callback_);
  device::BluetoothAdapter::Observer* observer =
      static_cast<device::BluetoothAdapter::Observer*>(&characteristic_finder);

  EXPECT_EQ(kToPeripheralCharID, found_to_char.id);
  EXPECT_EQ(kFromPeripheralCharID, found_from_char.id);

  EXPECT_CALL(*service_, GetUUID())
      .WillOnce(Return(device::BluetoothUUID(kServiceUUID)));
  observer->GattDiscoveryCompleteForService(adapter_.get(), service_.get());
}

}  // namespace cryptauth
